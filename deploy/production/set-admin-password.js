// SboSvr.ini の [Administrator] の Password= だけを書き換える（ほかの節の値には触らない）。
// start.ps1 が sbo2003.env の SBO2003_ADMIN_PASSWORD を渡して呼ぶ（ステージングの entrypoint.sh と同じ処理）。
//
//   node set-admin-password.js <SboSvr.ini> <パスワード>
'use strict';
const fs = require('fs');
const file = process.argv[2], pass = process.argv[3];
if (!file || pass === undefined) {
	console.error('使い方: node set-admin-password.js <SboSvr.ini> <パスワード>');
	process.exit(1);
}
let sect = '';
const out = fs.readFileSync(file, 'latin1').split(/\r?\n/).map((line) => {
	const m = line.match(/^\s*\[(.+)\]\s*$/);
	if (m) { sect = m[1].toLowerCase(); return line; }
	if (sect === 'administrator' && /^\s*Password\s*=/i.test(line)) { return 'Password=' + pass; }
	return line;
});
fs.writeFileSync(file, out.join('\r\n'), 'latin1');
