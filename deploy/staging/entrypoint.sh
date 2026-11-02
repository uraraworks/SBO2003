#!/bin/sh
# ステージングのコンテナの入口。
#   1. データフォルダ（ボリューム）が空なら、イメージに入れた元データをコピーする
#   2. 環境変数 SBO2003_ADMIN_PASSWORD があれば、SboSvr.ini の管理者パスワードにする
#   3. サーバーを起動する（node に置き換わるので、SIGTERM はそのままサーバーに届く）
set -eu

DATA=/srv/sbo2003/data
if [ ! -d "$DATA/SBODATA" ]; then
	echo "[entrypoint] データフォルダを作ります: $DATA"
	cp -R /srv/sbo2003/seed/SBODATA "$DATA/"
	cp /srv/sbo2003/seed/SboSvr.ini "$DATA/SboSvr.ini"
fi

if [ -n "${SBO2003_ADMIN_PASSWORD:-}" ]; then
	# [Administrator] の Password= だけを書き換える（ほかの節の値には触らない）
	node -e '
		const fs = require("fs");
		const file = process.argv[1], pass = process.argv[2];
		let sect = "";
		const out = fs.readFileSync(file, "latin1").split(/\r?\n/).map((line) => {
			const m = line.match(/^\s*\[(.+)\]\s*$/);
			if (m) { sect = m[1].toLowerCase(); return line; }
			if (sect === "administrator" && /^\s*Password\s*=/i.test(line)) { return "Password=" + pass; }
			return line;
		});
		fs.writeFileSync(file, out.join("\r\n"), "latin1");
	' "$DATA/SboSvr.ini" "$SBO2003_ADMIN_PASSWORD"
	echo "[entrypoint] 管理者パスワードを設定しました"
fi

exec node /srv/sbo2003/server/SboSvr.js --headless --data "$DATA" \
	--http-port 8080 --www /srv/sbo2003/web/dist
