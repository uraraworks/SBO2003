/* 利用環境の統計（server/node/sbosvr_stats.js）の単体テスト
 *   実行: node tests/server/test_stats.js    （「全部 OK」と出れば成功）
 *   User-Agent の分類と、記録・集計（accounts / main / logins / multi）・90 日整理を確かめる。
 */
var assert = require('assert');
var fs = require('fs');
var os = require('os');
var path = require('path');

var strDir = fs.mkdtempSync(path.join(os.tmpdir(), 'sbostats-'));
fs.mkdirSync(path.join(strDir, 'SBODATA'));
global.FS = { cwd: function () { return strDir; } };
global.Module = {};
require('../../server/node/sbosvr_stats.js');
var st = Module['sboStats'];

var nOk = 0;
function eq(a, b, msg) {
	assert.deepStrictEqual(a, b, msg);
	nOk++;
}

/* ---- User-Agent の分類 ---- */
var UA = {
	winChrome: 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36',
	winEdge: 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36 Edg/126.0.0.0',
	winFirefox: 'Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:127.0) Gecko/20100101 Firefox/127.0',
	winOpera: 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.0.0 Safari/537.36 OPR/111.0.0.0',
	macSafari: 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.5 Safari/605.1.15',
	macChrome: 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36',
	iphoneSafari: 'Mozilla/5.0 (iPhone; CPU iPhone OS 17_5 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.5 Mobile/15E148 Safari/604.1',
	iphoneChrome: 'Mozilla/5.0 (iPhone; CPU iPhone OS 17_5 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) CriOS/126.0.6478.153 Mobile/15E148 Safari/604.1',
	iphoneLine: 'Mozilla/5.0 (iPhone; CPU iPhone OS 17_5 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Mobile/15E148 Safari Line/14.9.0',
	ipad: 'Mozilla/5.0 (iPad; CPU OS 17_5 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.5 Mobile/15E148 Safari/604.1',
	androidChrome: 'Mozilla/5.0 (Linux; Android 14; Pixel 8) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Mobile Safari/537.36',
	androidTablet: 'Mozilla/5.0 (Linux; Android 13; SM-X710) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36',
	androidSamsung: 'Mozilla/5.0 (Linux; Android 14; SM-S918B) AppleWebKit/537.36 (KHTML, like Gecko) SamsungBrowser/25.0 Chrome/121.0.0.0 Mobile Safari/537.36',
	androidEdge: 'Mozilla/5.0 (Linux; Android 14; Pixel 8) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Mobile Safari/537.36 EdgA/126.0.0.0',
	androidFirefox: 'Mozilla/5.0 (Android 14; Mobile; rv:127.0) Gecko/127.0 Firefox/127.0',
	iosEdge: 'Mozilla/5.0 (iPhone; CPU iPhone OS 17_5 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.0 EdgiOS/126.2592.87 Mobile/15E148 Safari/605.1.15',
	linuxFirefox: 'Mozilla/5.0 (X11; Linux x86_64; rv:127.0) Gecko/20100101 Firefox/127.0',
	chromeos: 'Mozilla/5.0 (X11; CrOS x86_64 14541.0.0) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36'
};
function c(ua, tp) {
	var e = st.classifyEnv(ua, tp);
	return e.os + '|' + e.browser + '|' + e.device;
}
eq(c(UA.winChrome), 'windows|chrome|pc');
eq(c(UA.winEdge), 'windows|edge|pc');
eq(c(UA.winFirefox), 'windows|firefox|pc');
eq(c(UA.winOpera), 'windows|opera|pc');
eq(c(UA.macSafari), 'mac|safari|pc');
eq(c(UA.macChrome), 'mac|chrome|pc');
eq(c(UA.macSafari, true), 'ios|safari|tablet', 'iPadOS（Mac の UA + タッチ）');
eq(c(UA.iphoneSafari), 'ios|safari|phone');
eq(c(UA.iphoneChrome), 'ios|chrome|phone');
eq(c(UA.iphoneLine), 'ios|other|phone', 'LINE のアプリ内ブラウザ');
eq(c(UA.ipad), 'ios|safari|tablet');
eq(c(UA.androidChrome), 'android|chrome|phone');
eq(c(UA.androidTablet), 'android|chrome|tablet', 'Mobile が無い Android はタブレット');
eq(c(UA.androidSamsung), 'android|samsung|phone');
eq(c(UA.androidEdge), 'android|edge|phone');
eq(c(UA.androidFirefox), 'android|firefox|phone');
eq(c(UA.iosEdge), 'ios|edge|phone');
eq(c(UA.linuxFirefox), 'linux|firefox|pc');
eq(c(UA.chromeos), 'chromeos|chrome|pc');
eq(c(''), 'other|other|pc');
eq(c(undefined), 'other|other|pc');
var n = st.nativeEnv();
eq(n.os + '|' + n.browser + '|' + n.device, 'windows|native|pc');

/* ---- 記録と集計 ---- */
var DAY = 86400000;
var T0 = Date.UTC(2026, 5, 30, 3, 0, 0);		/* 2026-06-30 12:00 JST */
function env(os, browser, device) { return { os: os, browser: browser, device: device }; }
var WIN = env('windows', 'chrome', 'pc'), IPH = env('ios', 'safari', 'phone'), MAC = env('mac', 'safari', 'pc');

/* a: 今日 Windows 2 回 + iPhone 1 回（メインは Windows・PC とスマホ両方）、b: iPhone だけ 2 回、c: Mac 1 回 */
st.record('acc-a', WIN, T0);
st.record('acc-a', WIN, T0);
st.record('acc-a', IPH, T0);
st.record('acc-b', IPH, T0);
st.record('acc-b', IPH, T0 - 3 * DAY);
st.record('acc-c', MAC, T0 - 10 * DAY);
/* 100 日前の記録（整理で old へ移る） */
st.record('acc-a', WIN, T0 - 100 * DAY);
st.record('acc-d', IPH, T0 - 100 * DAY);

function row(list, key) {
	return list.filter(function (r) { return r.key === key; })[0];
}
var r1 = st.aggregate(1, T0);
eq([r1.total, r1.multi], [2, 1], '今日: 実人数 2（a, b）・両方使った人 1（a）');
eq(row(r1.axes.os, 'windows'), { key: 'windows', accounts: 1, main: 1, logins: 2 });
eq(row(r1.axes.os, 'ios'), { key: 'ios', accounts: 2, main: 1, logins: 2 }, 'a は両方に 1 だが main は Windows');
eq(row(r1.axes.device, 'phone').logins, 2);
eq(row(r1.axes.osBrowser, 'ios+safari').accounts, 2);

var r7 = st.aggregate(7, T0);
eq([r7.total, r7.multi], [2, 1]);
eq(row(r7.axes.os, 'ios').logins, 3, '3 日前の b も入る');
var r30 = st.aggregate(30, T0);
eq([r30.total, r30.multi], [3, 1]);
eq(r30.axes.os.reduce(function (s, r) { return s + r.main; }, 0), 3, 'main の合計 = 実人数');
eq(r30.axes.browser.reduce(function (s, r) { return s + r.main; }, 0), 3);
eq(row(r30.axes.os, 'mac'), { key: 'mac', accounts: 1, main: 1, logins: 1 });

/* 90 日整理: 100 日前の日はアカウントを捨てて old に回数だけ残る */
var r90 = st.aggregate(90, T0);
eq(r90.total, 3, '90 日の範囲には 100 日前は入らない');
var all = st.aggregate(0, T0);
eq([all.total, all.multi], [null, null]);
eq(row(all.axes.os, 'windows'), { key: 'windows', accounts: null, main: null, logins: 3 }, '全期間は延べ回数だけ（100 日前の 1 回を含む）');
eq(row(all.axes.os, 'ios').logins, 4);
eq(all.axes.os.reduce(function (s, r) { return s + r.logins; }, 0), 8, '延べ回数の合計 = 記録した回数');

st.flush();
var strFile = path.join(strDir, 'SBODATA', 'SboWebStats.json');
var json = fs.readFileSync(strFile, 'utf8');
var saved = JSON.parse(json);
eq(Object.keys(saved.old), ['2026-03-22'], '整理された日は old に残る');
eq(saved.old['2026-03-22'], { 'windows|chrome|pc': 1, 'ios|safari|phone': 1 });
eq(saved.days['2026-03-22'], undefined, '整理された日のアカウント別は残らない');
eq(json.indexOf('acc-a') < 0 && json.indexOf('acc-b') < 0, true, 'アカウント名はそのまま書かない');
eq(/^[0-9a-f]{32}$/.test(saved.salt), true);
Object.keys(saved.days).forEach(function (d) {
	Object.keys(saved.days[d]).forEach(function (h) { eq(/^[0-9a-f]{16}$/.test(h), true); });
});
eq(Object.keys(saved.days).length, 3);

/* 日付は日本時間（UTC 15:00 で日が変わる） */
st.record('acc-e', WIN, Date.UTC(2026, 5, 30, 14, 59, 0));
st.record('acc-e', WIN, Date.UTC(2026, 5, 30, 15, 0, 0));
eq(Object.keys(JSON.parse((st.flush(), fs.readFileSync(strFile, 'utf8'))).days).indexOf('2026-07-01') >= 0, true);

fs.rmSync(strDir, { recursive: true, force: true });
console.log('全部 OK（' + nOk + ' 件）');
