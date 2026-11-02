#!/usr/bin/env bash
# Sbo2003 のステージングを最新にする（ステージングのホストで実行。GitHub Actions からも呼ぶ）。
#
#   deploy/staging/deploy.sh --image <sbo2003-staging.tar.gz>   Actions で作ったイメージを読み込んで入れ替える
#   deploy/staging/deploy.sh                                     このホストでイメージを作って入れ替える
#
# SBOP2 のステージング（Caddy）が同じホストで動いていることが前提。
# このディレクトリに staging.env が必要。
set -euo pipefail

CADDY=sbop2-staging-caddy-1
IMAGE_TAR=""
if [ "${1:-}" = "--image" ]; then
	IMAGE_TAR="${2:?--image にはイメージのファイルを指定してください}"
	# 下で cd するので、先に絶対パスにしておく
	IMAGE_TAR="$(cd "$(dirname "$IMAGE_TAR")" && pwd)/$(basename "$IMAGE_TAR")"
fi
cd "$(dirname "$0")"
[ -f staging.env ] || { echo "staging.env がありません（staging.env.example をコピーして作成）" >&2; exit 1; }

# staging.env から値を読む（source すると bcrypt ハッシュの $ が展開されるので、行をそのまま切り出す）
env_get() {
	sed -n "s/^$1=//p" staging.env | tail -n 1 | tr -d '\r'
}
DOMAIN="$(env_get SBO2003_STAGING_DOMAIN)"
BASIC_USER="$(env_get SBO2003_BASIC_USER)"
BASIC_HASH="$(env_get SBO2003_BASIC_HASH)"
[ -n "$DOMAIN" ] && [ -n "$BASIC_USER" ] && [ -n "$BASIC_HASH" ] || {
	echo "staging.env の SBO2003_STAGING_DOMAIN / SBO2003_BASIC_USER / SBO2003_BASIC_HASH を埋めてください" >&2
	exit 1
}
[ -n "$(env_get SBO2003_ADMIN_PASSWORD)" ] || echo "[deploy] 注意: SBO2003_ADMIN_PASSWORD が空です（管理者パスワード無しのまま）" >&2

docker inspect "$CADDY" >/dev/null 2>&1 || {
	echo "SBOP2 のステージングの Caddy（$CADDY）が見つかりません。先に SBOP2 のステージングを用意してください" >&2
	exit 1
}

if [ -n "$IMAGE_TAR" ]; then
	echo "[deploy] イメージを読み込みます: $IMAGE_TAR"
	gzip -dc "$IMAGE_TAR" | docker load
else
	docker compose build sbosvr
fi

# 共有ネットワーク（SBOP2 の deploy.sh も作るが、念のため）
docker network create staging-shared >/dev/null 2>&1 || true
mkdir -p "$HOME/caddy-sites"

# サーバーを新しいイメージで作り直す（SIGTERM でデータを保存してから止まる）
docker compose up -d
docker image prune -f >/dev/null

# 起動確認（Caddy を通さず、共有ネットワーク越しに直接叩く）
ok=""
for _ in $(seq 1 30); do
	if docker exec "$CADDY" wget -q -O - http://sbo2003-staging:8080/health 2>/dev/null; then
		ok=1
		break
	fi
	sleep 2
done
if [ -z "$ok" ]; then
	echo "[deploy] /health に応答がありません" >&2
	docker compose logs --tail 50 sbosvr >&2
	exit 1
fi

# サイト定義を置いて Caddy に読み直させる（中身が変わった時だけ）
SITE="$HOME/caddy-sites/sbo2003.caddy"
TMP="$(mktemp)"
# 置換文字列の & と | と \ をエスケープしてから埋める
esc() { printf '%s' "$1" | sed -e 's/[&|\\]/\\&/g'; }
sed -e "s|@@DOMAIN@@|$(esc "$DOMAIN")|" \
	-e "s|@@BASIC_USER@@|$(esc "$BASIC_USER")|" \
	-e "s|@@BASIC_HASH@@|$(esc "$BASIC_HASH")|" \
	sbo2003.caddy.template > "$TMP"
if cmp -s "$TMP" "$SITE"; then
	rm -f "$TMP"
	echo "[deploy] サイト定義は変わっていません"
else
	OLD="$(mktemp)"
	[ -f "$SITE" ] && cp "$SITE" "$OLD" || : > "$OLD"
	mv "$TMP" "$SITE"
	chmod 644 "$SITE"
	if docker exec "$CADDY" caddy reload --config /etc/caddy/Caddyfile; then
		echo "[deploy] サイト定義を更新しました: https://$DOMAIN/"
		rm -f "$OLD"
	else
		# 読み直しに失敗したら元に戻す（Caddy は前の設定のまま動き続けている）
		echo "[deploy] Caddy の読み直しに失敗したので、サイト定義を元に戻します" >&2
		if [ -s "$OLD" ]; then mv "$OLD" "$SITE"; else rm -f "$SITE" "$OLD"; fi
		exit 1
	fi
fi
echo "[deploy] 起動を確認しました"
