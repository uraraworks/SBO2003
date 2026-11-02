#!/usr/bin/env bash
# Sbo2003 を本番 VPS（Windows Server + OpenSSH）へ SSH で送って入れ替える。
# Actions の production-publish から呼ばれる。Mac から手で動かしてもよい。
#
#   deploy/production/vps-deploy.sh <zip>              # 新しい版を入れる
#   deploy/production/vps-deploy.sh --data <zip>       # データだけ入れ替える（SBODATA/ と SboSvr.ini の zip）（move-staging-data.sh から）
#   deploy/production/vps-deploy.sh --rollback         # 1 つ前の版に戻す
#
# 接続先は環境変数で渡す（Mac では deploy/production/deploy.local.env に書いておくと読み込む）:
#   VPS_HOST   VPS のホスト名か IP（必須）
#   VPS_USER   SSH のユーザー（Administrators に入っていること。必須）
#   VPS_PORT   SSH のポート（既定 22）
#   SSH_KEY    秘密鍵のファイル（既定 ~/.ssh/urara_vps_deploy）
#   VPS_ROOT   VPS 上の置き場所（既定 C:/SBO2003）
#   HEALTH_URL 外から確かめる URL（任意。例 https://sbo2003.urara-works.jp/health）
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
if [ -f "$HERE/deploy.local.env" ]; then
	# shellcheck disable=SC1091
	. "$HERE/deploy.local.env"
fi

ZIP="" DATA="" ROLLBACK=0
case "${1:-}" in
	--data) DATA="${2:?データの zip を指定してください}" ;;
	--rollback) ROLLBACK=1 ;;
	""|-*) echo "使い方: $0 <zip> | --data <zip> | --rollback" >&2; exit 2 ;;
	*) ZIP="$1" ;;
esac

: "${VPS_HOST:?VPS_HOST を指定してください}"
: "${VPS_USER:?VPS_USER を指定してください}"
VPS_PORT="${VPS_PORT:-22}"
SSH_KEY="${SSH_KEY:-$HOME/.ssh/urara_vps_deploy}"
SSH_KEY="${SSH_KEY/#\~/$HOME}"
VPS_ROOT="${VPS_ROOT:-C:/SBO2003}"
WIN_ROOT="${VPS_ROOT//\//\\}"
TARGET="$VPS_USER@$VPS_HOST"
SSH_OPTS=(-i "$SSH_KEY" -p "$VPS_PORT" -o BatchMode=yes -o ServerAliveInterval=30)
SCP_OPTS=(-i "$SSH_KEY" -P "$VPS_PORT" -o BatchMode=yes)

# VPS で powershell を動かす（SSH の既定のシェルが cmd でも PowerShell でも同じに動くよう、
# 引数には空白や引用符を含めない。パスにも空白を使わないこと）
remote() {
	ssh "${SSH_OPTS[@]}" "$TARGET" "powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass $*"
}
# 複数のコマンドは UTF-16LE + base64 にして -EncodedCommand で渡す（引用符の解釈の違いを避ける）
remote_cmd() {
	remote "-EncodedCommand $(printf '%s' "$1" | iconv -f UTF-8 -t UTF-16LE | base64 | tr -d '\n')"
}
case "$WIN_ROOT" in *" "*) echo "VPS_ROOT に空白は使えません: $VPS_ROOT" >&2; exit 2 ;; esac

echo "==> 受け取り場所を用意: $VPS_ROOT/upload"
remote_cmd "New-Item -ItemType Directory -Force -Path '$WIN_ROOT\\upload' | Out-Null"

# 入れ替えのスクリプトは毎回送る（初回は VPS にまだ無いため。BOM 付き UTF-8・CRLF にする）
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
{ printf '\xEF\xBB\xBF'; sed 's/$/\r/' "$HERE/remote-deploy.ps1"; } > "$TMP/remote-deploy.ps1"
FILES=("$TMP/remote-deploy.ps1")
ARGS=""
if [ -n "$ZIP" ]; then
	FILES+=("$ZIP")
	ARGS="-Zip $WIN_ROOT\\upload\\$(basename "$ZIP")"
elif [ -n "$DATA" ]; then
	FILES+=("$DATA")
	ARGS="-Data $WIN_ROOT\\upload\\$(basename "$DATA")"
else
	ARGS="-Rollback"
fi

echo "==> 転送（scp）: $(for f in "${FILES[@]}"; do basename "$f"; done | tr '\n' ' ')"
scp "${SCP_OPTS[@]}" "${FILES[@]}" "$TARGET:$VPS_ROOT/upload/"

echo "==> VPS で入れ替え"
remote "-File $WIN_ROOT\\upload\\remote-deploy.ps1 -Root $WIN_ROOT $ARGS"

if [ -n "${HEALTH_URL:-}" ]; then
	echo "==> 外から確かめる: $HEALTH_URL"
	for i in $(seq 1 10); do
		if curl -fsS --max-time 15 "$HEALTH_URL" | grep -q ok; then echo "OK"; exit 0; fi
		echo "   待っています（$i/10）"; sleep 5
	done
	echo "NG: $HEALTH_URL が ok を返しません（IIS のサイト・DNS・証明書を確かめてください）" >&2
	exit 1
fi
echo "==> 完了"
