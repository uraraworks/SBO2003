#!/usr/bin/env bash
# Sbo2003 を本番 VPS（Windows Server + OpenSSH）へ SSH で送って入れ替える。
# Actions の production-publish から呼ばれる。Mac から手で動かしてもよい。
#
#   deploy/production/vps-deploy.sh <zip>              # 新しい版を入れる
#   deploy/production/vps-deploy.sh --data <zip>       # データだけ入れ替える（SBODATA/ と SboSvr.ini の zip）（move-staging-data.sh から）
#   deploy/production/vps-deploy.sh --rollback         # 1 つ前の版に戻す
#   deploy/production/vps-deploy.sh --notice <文> [<文>...]  # ゲームの全員にお知らせを流すだけ（1 つが 1 行）
#                                                      # （Actions がビルドを始める時の再起動予告・中止の知らせに使う）
#
# 接続先は環境変数で渡す（Mac では deploy/production/deploy.local.env に書いておくと読み込む）:
#   VPS_HOST   VPS のホスト名か IP（必須）
#   VPS_USER   SSH のユーザー（Administrators に入っていること。必須）
#   VPS_PORT   SSH のポート（既定 22）
#   SSH_KEY    秘密鍵のファイル（既定 ~/.ssh/urara_vps_deploy）
#   VPS_ROOT   VPS 上の置き場所（既定 C:/SBO2003）
#   HEALTH_URL 外から確かめる URL（任意。例 https://sbo2003.urara-works.jp/health）
#   IIS_HOST   IIS のサイトのホスト名（任意。例 sbo2003.urara-works.jp）。指定すると毎回 setup-iis.ps1 で
#              サイトとバインドを確かめ、足りないものだけ足す（既存のサイトには触らない）
#   NOTICE_SEC 止める前にゲームの全員へ再起動を予告して待つ秒数（既定 60、0 で予告しない。
#              ゲームに入っている人がいない時は待たない）
#   NOTICE_ANNOUNCED=1  予告は --notice で済んでいる。止める前は「まもなく再起動します」だけ流して 5 秒後に止める
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
if [ -f "$HERE/deploy.local.env" ]; then
	# shellcheck disable=SC1091
	. "$HERE/deploy.local.env"
fi

ZIP="" DATA="" ROLLBACK=0 NOTICE=0
case "${1:-}" in
	--data) DATA="${2:?データの zip を指定してください}" ;;
	--rollback) ROLLBACK=1 ;;
	--notice) NOTICE=1; shift; [ $# -gt 0 ] || { echo "--notice には流す文を指定してください" >&2; exit 2; } ;;
	""|-*) echo "使い方: $0 <zip> | --data <zip> | --rollback | --notice <文>..." >&2; exit 2 ;;
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

if [ "$NOTICE" = 1 ]; then
	# お知らせファイル（data\notice.request）を置くと、サーバーが全員に流して消す（server/node/sbosvr_net.js）。
	# サーバーが止まっている時は置かない（次の起動で start.ps1 が消す）
	lines=""
	for l in "$@"; do lines="$lines,'${l//\'/\'\'}'"; done
	echo "==> お知らせを流す: $*"
	remote_cmd "\$ErrorActionPreference = 'Stop'
\$f = '$WIN_ROOT\\data\\notice.request'
\$pidFile = '$WIN_ROOT\\data\\sbosvr.pid'
if (-not (Test-Path \$pidFile) -or -not (Get-Process -Id ([int](Get-Content \$pidFile -Raw)) -ErrorAction SilentlyContinue)) {
	Write-Output 'サーバーが動いていないので流しません'; exit 0
}
[IO.File]::WriteAllText(\"\$f.tmp\", (@(${lines#,}) -join \"\`r\`n\"), (New-Object Text.UTF8Encoding \$false))
Move-Item -Force \"\$f.tmp\" \$f
for (\$i = 0; \$i -lt 10 -and (Test-Path \$f); \$i++) { Start-Sleep -Milliseconds 500 }
if (Test-Path \$f) { Write-Output 'サーバーがお知らせを読みませんでした（古い版？）'; Remove-Item -Force \$f } else { Write-Output '流しました' }"
	exit 0
fi

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
NOTICE_SEC="${NOTICE_SEC:-60}"
case "$NOTICE_SEC" in ''|*[!0-9]*) echo "NOTICE_SEC は 0 以上の整数で指定してください: $NOTICE_SEC" >&2; exit 2 ;; esac
ARGS="$ARGS -NoticeSec $NOTICE_SEC"
if [ "${NOTICE_ANNOUNCED:-}" = 1 ]; then ARGS="$ARGS -Announced"; fi

echo "==> 転送（scp）: $(for f in "${FILES[@]}"; do basename "$f"; done | tr '\n' ' ')"
scp "${SCP_OPTS[@]}" "${FILES[@]}" "$TARGET:$VPS_ROOT/upload/"

echo "==> VPS で入れ替え"
remote "-File $WIN_ROOT\\upload\\remote-deploy.ps1 -Root $WIN_ROOT $ARGS"

if [ -n "${IIS_HOST:-}" ]; then
	echo "==> IIS のサイトを確かめる: $IIS_HOST"
	remote "-File $WIN_ROOT\\setup-iis.ps1 -Root $WIN_ROOT -HostName $IIS_HOST"
fi

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
