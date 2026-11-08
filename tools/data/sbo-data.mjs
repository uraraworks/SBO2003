#!/usr/bin/env node
// ===========================================================================
// サーバーデータの JSON 書き出し・見比べ・取り込み（データ API を使う）
//
//   node tools/data/sbo-data.mjs export [--dir DIR] [--kinds a,b]   サーバー → JSON ファイル
//   node tools/data/sbo-data.mjs diff   [--dir DIR] [--kinds a,b]   JSON ファイルとサーバーの違いを見る（何も変えない）
//   node tools/data/sbo-data.mjs import [--dir DIR] [--kinds a,b] [--yes] [--note 文]
//                                                                  JSON ファイルの中身をサーバーへ（下見 → 確認 → 反映）
//
// 接続先（どれか 1 つ）:
//   --target prod      本番     https://sbo2003.urara-works.jp            キーは SBO2003_DATA_KEY か SBO2003_AI_KEY
//   --target staging   ステージング https://sbo2003.136-109-105-232.sslip.io キーは SBO2003_DATA_KEY か SBO2003_STAGING_AI_KEY
//   --url URL          それ以外（ローカルなら http://localhost:8080 など）  キーは SBO2003_DATA_KEY
//   環境変数 SBO2003_DATA_URL を決めておけば --target / --url は要らない。
//   キーは --key-file ファイル でも渡せる（中身の 1 行目）。書き出し・見比べは読むだけのキーで足りる。
//
// JSON の置き場所（--dir を付けない時）: 非公開リポジトリの data/
//   （tools/server/find-data.sh と同じ探し方。SBO2003_PRIVATE_DIR か、このリポジトリと同じ階層の SBO2003-private）
//
// 取り込みのきまり:
//   - 書き換えられる種類（magic・magicUser・equUser・job・stTmp・item・maker・recipe）だけ反映する。
//     読むだけの種類（map など）は違いを出すだけ。
//   - 番号（id）で突き合わせ、違う項目だけを update で送る。サーバーより大きい番号は add で足す（番号は続きで）。
//   - ファイルから消した記録は、サーバーからは消さない（maker・recipe の削除は byState を 0 に）。
//   - 反映は AI 連携の変更履歴に残り、管理ページ「AI連携」や /api/data/undo から元に戻せる（追加は戻せない）。
// ===========================================================================
import fs from 'node:fs';
import path from 'node:path';
import readline from 'node:readline';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const ROOT = path.resolve(HERE, '..', '..');

const TARGETS = {
	prod: { url: 'https://sbo2003.urara-works.jp', keyEnv: 'SBO2003_AI_KEY' },
	staging: { url: 'https://sbo2003.136-109-105-232.sslip.io', keyEnv: 'SBO2003_STAGING_AI_KEY' },
};

/* 書き出す種類。char（プレイヤーが混ざる・番号が動く）と leave（プレイヤーの預かり所）は出さない */
const EXPORT_KINDS = ['magic', 'magicUser', 'equUser', 'job', 'stTmp', 'item', 'maker', 'recipe', 'map', 'mapParts', 'efc', 'comEfc', 'subGame'];
const SKIP_KINDS = { char: 'プレイヤーが混ざり、番号も動くので書き出しません', leave: 'プレイヤーの預かり所なので書き出しません' };

const MAX_ITEMS = 200;			/* API が 1 回で受け付ける件数 */
const MAX_BYTES = 200 * 1024;	/* API の上限（256KB）より少し小さく */

/* ------------------------------------------------------------------ */
/* 引数																	 */
/* ------------------------------------------------------------------ */
function parseArgs(argv) {
	const a = { cmd: argv[0], flags: {} };
	for (let i = 1; i < argv.length; i++) {
		const s = argv[i];
		if (!s.startsWith('--')) {
			die('分からない引数です: ' + s);
		}
		const name = s.slice(2);
		if (name === 'yes' || name === 'help') {
			a.flags[name] = true;
		} else {
			if (i + 1 >= argv.length) {
				die('--' + name + ' の後に値を書いてください');
			}
			a.flags[name] = argv[++i];
		}
	}
	return a;
}

function die(msg) {
	console.error('エラー: ' + msg);
	process.exit(1);
}

function usage() {
	const src = fs.readFileSync(fileURLToPath(import.meta.url), 'utf8').split('\n');
	const aOut = [];
	for (const line of src.slice(1)) {
		if (!line.startsWith('//')) {
			break;
		}
		aOut.push(line.replace(/^\/\/ ?/, ''));
	}
	console.log(aOut.join('\n'));
}

/* 接続先とキー */
function connection(flags) {
	let url = flags.url || '';
	let keyEnv = '';
	if (flags.target) {
		const t = TARGETS[flags.target];
		if (!t) {
			die('--target は prod か staging です');
		}
		url = t.url;
		keyEnv = t.keyEnv;
	}
	if (!url) {
		url = process.env.SBO2003_DATA_URL || '';
	}
	if (!url) {
		die('接続先がありません。--target prod / --target staging / --url URL のどれかを付けてください');
	}
	let key = '';
	if (flags['key-file']) {
		key = fs.readFileSync(flags['key-file'], 'utf8').split('\n')[0].trim();
	} else {
		key = process.env.SBO2003_DATA_KEY || (keyEnv ? process.env[keyEnv] : '') || '';
	}
	if (!key) {
		die('AI 用キーがありません。環境変数 SBO2003_DATA_KEY か --key-file で渡してください（管理ページ /admin の「AI連携」で発行）');
	}
	return { url: url.replace(/\/+$/, ''), key };
}

/* JSON の置き場所 */
function dataDir(flags) {
	if (flags.dir) {
		return path.resolve(flags.dir);
	}
	let repo = '';
	try {
		repo = execFileSync(path.join(ROOT, 'tools', 'server', 'find-data.sh'), ['--repo'], { encoding: 'utf8' }).trim();
	} catch (e) {
		repo = '';
	}
	if (!repo) {
		die('非公開リポジトリ（SBO2003-private）が見つかりません。--dir で置き場所を指定するか、SBO2003_PRIVATE_DIR を設定してください');
	}
	return path.join(repo, 'data');
}

/* ------------------------------------------------------------------ */
/* API																	 */
/* ------------------------------------------------------------------ */
async function api(conn, name, body) {
	const res = await fetch(conn.url + '/api/data/' + name, {
		method: 'POST',
		headers: { 'Authorization': 'Bearer ' + conn.key, 'Content-Type': 'application/json' },
		body: JSON.stringify(body),
	});
	const text = await res.text();
	let obj;
	try {
		obj = JSON.parse(text);
	} catch (e) {
		throw new Error(name + ': サーバーの返事が JSON ではありません（HTTP ' + res.status + '）: ' + text.slice(0, 200));
	}
	if (!res.ok || obj.error) {
		const msg = obj.message || obj.error || ('HTTP ' + res.status);
		throw new Error(name + ': ' + (typeof msg === 'string' ? msg : JSON.stringify(msg)));
	}
	return obj;
}

async function listAll(conn, kind) {
	const aOut = [];
	let offset = 0;
	for (;;) {
		const r = await api(conn, 'list', { kind, offset, limit: 500 });
		aOut.push(...r.records);
		if (!r.more || r.records.length === 0) {
			break;
		}
		offset += r.records.length;
	}
	return aOut;
}

/* ------------------------------------------------------------------ */
/* 見比べ・整形															 */
/* ------------------------------------------------------------------ */
/* キーの順番によらず同じかどうか */
function canon(v) {
	if (Array.isArray(v)) {
		return '[' + v.map(canon).join(',') + ']';
	}
	if (v && typeof v === 'object') {
		return '{' + Object.keys(v).sort().map((k) => JSON.stringify(k) + ':' + canon(v[k])).join(',') + '}';
	}
	return JSON.stringify(v);
}
function same(a, b) {
	return canon(a) === canon(b);
}

/* git で見比べやすい形: 1 記録 = 項目ごとに 1 行、オブジェクトの配列は 1 要素 1 行 */
function formatRecord(rec, ind) {
	const aKey = Object.keys(rec);
	const aLine = aKey.map((k) => {
		const v = rec[k];
		let s;
		if (Array.isArray(v) && v.length && v.every((x) => x && typeof x === 'object' && !Array.isArray(x))) {
			s = '[\n' + v.map((x) => ind + '\t\t' + JSON.stringify(x)).join(',\n') + '\n' + ind + '\t]';
		} else {
			s = JSON.stringify(v);
		}
		return ind + '\t' + JSON.stringify(k) + ': ' + s;
	});
	return ind + '{\n' + aLine.join(',\n') + '\n' + ind + '}';
}

function formatFile(kind, info, records) {
	const head = {
		kind,
		label: info ? info.label : '',
		note: (info && info.readonly) ? '読むだけ（参考用）。書き換えても取り込まれません' : '番号（id）で突き合わせて取り込みます。id より後ろの項目を書き換えてください',
	};
	let s = '{\n';
	for (const k of Object.keys(head)) {
		s += '\t' + JSON.stringify(k) + ': ' + JSON.stringify(head[k]) + ',\n';
	}
	s += '\t"records": [\n' + records.map((r) => formatRecord(r, '\t\t')).join(',\n') + (records.length ? '\n' : '') + '\t]\n}\n';
	return s;
}

function readKindFile(dir, kind) {
	const file = path.join(dir, kind + '.json');
	if (!fs.existsSync(file)) {
		return null;
	}
	let obj;
	try {
		obj = JSON.parse(fs.readFileSync(file, 'utf8'));
	} catch (e) {
		die(file + ' が JSON として読めません: ' + e.message);
	}
	if (!obj || !Array.isArray(obj.records)) {
		die(file + ' に records の配列がありません');
	}
	const aSeen = {};
	for (const r of obj.records) {
		if (!r || typeof r !== 'object' || !Number.isInteger(r.id) || r.id < 0) {
			die(file + ': id（0 以上の整数）が無い記録があります');
		}
		if (aSeen[r.id]) {
			die(file + ': id ' + r.id + ' が 2 回あります');
		}
		aSeen[r.id] = 1;
	}
	return obj.records;
}

function short(v) {
	const s = JSON.stringify(v);
	return s === undefined ? '（無し）' : (s.length > 120 ? s.slice(0, 117) + '...' : s);
}

/* ファイルとサーバーを比べて、送る操作を作る */
function plan(info, fileRecs, srvRecs) {
	const aRo = { id: 1 };
	for (const f of (info.fields || [])) {
		if (f.readonly) {
			aRo[f.name] = 1;
		}
	}
	const srvById = new Map(srvRecs.map((r) => [r.id, r]));
	const total = info.count;
	const updates = [], adds = [], notes = [];
	const sorted = [...fileRecs].sort((a, b) => a.id - b.id);
	let nextAdd = total;
	for (const r of sorted) {
		const changes = {};
		for (const k of Object.keys(r)) {
			if (!aRo[k]) {
				changes[k] = r[k];
			}
		}
		if (r.id < total) {
			const s = srvById.get(r.id);
			if (!s) {
				notes.push('id ' + r.id + ' はサーバーで読めませんでした（飛ばします）');
				continue;
			}
			const diff = {};
			for (const k of Object.keys(changes)) {
				if (!same(changes[k], s[k])) {
					diff[k] = changes[k];
				}
			}
			for (const k of Object.keys(r)) {
				if (aRo[k] && k !== 'id' && !same(r[k], s[k])) {
					notes.push('id ' + r.id + ' の ' + k + ' は書き換えられない項目なので取り込みません');
				}
			}
			if (Object.keys(diff).length) {
				updates.push({ id: r.id, name: s.szName, changes: diff, before: s });
			}
		} else {
			if (r.id !== nextAdd) {
				die(info.kind + ': 新しく足す記録の id は ' + nextAdd + ' からの続き番号にしてください（' + r.id + ' があります）');
			}
			nextAdd++;
			adds.push({ id: r.id, name: r.szName, record: changes });
		}
	}
	const aInFile = new Set(fileRecs.map((r) => r.id));
	const nMissing = srvRecs.filter((r) => !aInFile.has(r.id)).length;
	if (nMissing) {
		notes.push('ファイルに無い記録が ' + nMissing + ' 件あります（サーバーからは消しません）');
	}
	return { updates, adds, notes };
}

function printPlan(kind, p) {
	if (!p.updates.length && !p.adds.length) {
		console.log('  ' + kind + ': 違いはありません');
	}
	for (const u of p.updates) {
		console.log('  ' + kind + ' ' + u.id + (u.name ? '「' + u.name + '」' : '') + ' を変更');
		for (const k of Object.keys(u.changes)) {
			console.log('      ' + k + ': ' + short(u.before[k]) + ' → ' + short(u.changes[k]));
		}
	}
	for (const a of p.adds) {
		console.log('  ' + kind + ' ' + a.id + (a.name ? '「' + a.name + '」' : '') + ' を追加: ' + short(a.record));
	}
	for (const n of p.notes) {
		console.log('  ' + kind + ': ※' + n);
	}
}

/* 件数と大きさの上限に収まるように分ける */
function chunks(list, toJson) {
	const aOut = [];
	let cur = [], nBytes = 0;
	for (const x of list) {
		const n = Buffer.byteLength(JSON.stringify(toJson(x)));
		if (cur.length && (cur.length >= MAX_ITEMS || nBytes + n > MAX_BYTES)) {
			aOut.push(cur);
			cur = [];
			nBytes = 0;
		}
		cur.push(x);
		nBytes += n;
	}
	if (cur.length) {
		aOut.push(cur);
	}
	return aOut;
}

function ask(q) {
	const rl = readline.createInterface({ input: process.stdin, output: process.stdout });
	return new Promise((resolve) => {
		let bDone = false;
		rl.on('close', () => { if (!bDone) { bDone = true; console.log(''); resolve(''); } });
		rl.question(q, (ans) => { bDone = true; rl.close(); resolve(ans.trim()); });
	});
}

function kindsFrom(flags, schema, def) {
	const all = schema.kinds.map((k) => k.kind);
	if (!flags.kinds) {
		return def;
	}
	const a = flags.kinds.split(',').map((s) => s.trim()).filter(Boolean);
	for (const k of a) {
		if (SKIP_KINDS[k]) {
			die(k + ' は' + SKIP_KINDS[k]);
		}
		if (!all.includes(k)) {
			die('知らない種類です: ' + k + '（' + all.join(', ') + '）');
		}
	}
	return a;
}

/* ------------------------------------------------------------------ */
/* コマンド																 */
/* ------------------------------------------------------------------ */
async function cmdExport(flags) {
	const conn = connection(flags);
	const dir = dataDir(flags);
	const schema = await api(conn, 'schema', {});
	const kinds = kindsFrom(flags, schema, EXPORT_KINDS);
	fs.mkdirSync(dir, { recursive: true });
	for (const kind of kinds) {
		const info = schema.kinds.find((k) => k.kind === kind);
		const recs = await listAll(conn, kind);
		fs.writeFileSync(path.join(dir, kind + '.json'), formatFile(kind, info, recs));
		console.log('  ' + kind + ': ' + recs.length + ' 件');
	}
	console.log(conn.url + ' から ' + dir + ' へ書き出しました');
}

async function compareAll(conn, flags, dir) {
	const schema = await api(conn, 'schema', {});
	const def = EXPORT_KINDS.filter((k) => fs.existsSync(path.join(dir, k + '.json')));
	const kinds = kindsFrom(flags, schema, def);
	const aOut = [];
	for (const kind of kinds) {
		const info = schema.kinds.find((k) => k.kind === kind);
		const fileRecs = readKindFile(dir, kind);
		if (!fileRecs) {
			console.log('  ' + kind + ': ファイルがありません（飛ばします）');
			continue;
		}
		const srvRecs = await listAll(conn, kind);
		const p = plan(info, fileRecs, srvRecs);
		if (info.readonly) {
			if (p.updates.length || p.adds.length) {
				console.log('  ' + kind + ': 読むだけの種類なので取り込みません（違い ' + (p.updates.length + p.adds.length) + ' 件）');
				printPlan(kind, p);
			} else {
				console.log('  ' + kind + ': 違いはありません');
			}
			continue;
		}
		printPlan(kind, p);
		aOut.push({ kind, plan: p });
	}
	return aOut;
}

async function cmdDiff(flags) {
	const conn = connection(flags);
	const dir = dataDir(flags);
	console.log(dir + ' と ' + conn.url + ' を見比べます（何も変えません）');
	await compareAll(conn, flags, dir);
}

async function cmdImport(flags) {
	const conn = connection(flags);
	const dir = dataDir(flags);
	const note = flags.note || 'JSON から取り込み（tools/data/sbo-data.mjs）';
	console.log(dir + ' を ' + conn.url + ' へ取り込みます。まず違いを見ます');
	const aKind = (await compareAll(conn, flags, dir)).filter((x) => x.plan.updates.length || x.plan.adds.length);
	if (!aKind.length) {
		console.log('取り込む違いはありません');
		return;
	}
	/* サーバーで下見（ここではまだ何も変わらない。中身の確かめもサーバーがする） */
	const aReq = [];
	for (const { kind, plan: p } of aKind) {
		for (const c of chunks(p.updates, (u) => ({ id: u.id, changes: u.changes }))) {
			aReq.push({ name: 'update', body: { kind, note, updates: c.map((u) => ({ id: u.id, changes: u.changes })) } });
		}
		if (p.adds.length) {
			/* 追加は件数の確かめがあるので 1 回にまとめる */
			if (p.adds.length > MAX_ITEMS) {
				die(kind + ': 1 回に足せるのは ' + MAX_ITEMS + ' 件までです');
			}
			aReq.push({ name: 'add', body: { kind, note, records: p.adds.map((a) => a.record) } });
		}
	}
	for (const r of aReq) {
		const pv = await api(conn, r.name, r.body);
		if (!pv.previewId) {
			die('下見の返事に previewId がありません');
		}
		r.previewId = pv.previewId;
	}
	const n = aKind.reduce((s, x) => s + x.plan.updates.length + x.plan.adds.length, 0);
	console.log('サーバーでの下見は通りました（' + n + ' 件、' + aReq.length + ' 回に分けて送ります）');
	if (!flags.yes) {
		const ans = await ask('反映しますか？ ログイン中のプレイヤーにもすぐ届きます [y/N] ');
		if (!/^y(es)?$/i.test(ans)) {
			console.log('やめました（何も変えていません）');
			return;
		}
	}
	for (const r of aReq) {
		const done = await api(conn, r.name, { ...r.body, previewId: r.previewId });
		console.log('  ' + r.body.kind + ' を' + (r.name === 'add' ? '追加' : '変更') + ' ' + done.items.length + ' 件（履歴 ' + done.historyId + '）');
	}
	console.log('反映しました。元に戻す時は管理ページ「AI連携」の変更履歴から');
}

/* ------------------------------------------------------------------ */
const args = parseArgs(process.argv.slice(2));
const CMDS = { export: cmdExport, diff: cmdDiff, import: cmdImport };
if (!args.cmd || args.flags.help || !CMDS[args.cmd]) {
	usage();
	process.exit(args.cmd && !args.flags.help && !CMDS[args.cmd] ? 1 : 0);
}
CMDS[args.cmd](args.flags).catch((e) => die(e.message));
