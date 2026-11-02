#!/bin/bash
# ===========================================================================
# サーバー（Node.js 版）を起動する
#
#   使い方: tools/server/run.sh [SboSvr.js に渡す引数...]
#     例: tools/server/run.sh --http-port 8081
#
# データフォルダ（out/server-data）が無ければ作ってから起動する。
# Ctrl+C か tools/server/stop.sh で、データを保存してから終了する。
# ===========================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
DATA="${SBO_DATA:-$ROOT/out/server-data}"
JS="$ROOT/out/server/SboSvr.js"

if [ ! -f "$JS" ]; then
	echo "[server] $JS がありません。先に tools/server/build.sh でビルドしてください" >&2
	exit 1
fi
if ! command -v node >/dev/null 2>&1; then
	echo "[server] node が見つかりません（Homebrew なら brew install node）" >&2
	exit 1
fi
"$ROOT/tools/server/setup-data.sh" "$DATA"

# stop.sh が止められるように、プロセス番号を残してから node に置き換わる
echo $$ > "$DATA/SboSvr.pid"
exec node "$JS" --headless --data "$DATA" ${1+"$@"}
