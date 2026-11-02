#!/bin/bash
# ===========================================================================
# サーバーの実行用フォルダを用意する
#
#   使い方: tools/server/setup-data.sh [フォルダ]   （省略時は out/server-data）
#
# Release/SBODATA と Release/sbosvr.ini をコピーする。すでにあれば何もしない
# （遊んだデータを上書きしないため。作り直す時はフォルダを消してから実行する）。
# SboSvr は「実行ファイル名.ini」を読むので、SboSvr.ini という名前で置く。
# ===========================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
DATA="${1:-$ROOT/out/server-data}"

if [ -d "$DATA/SBODATA" ]; then
	echo "[server] データフォルダはもうあります: $DATA"
	exit 0
fi
mkdir -p "$DATA"
cp -R "$ROOT/Release/SBODATA" "$DATA/"
cp "$ROOT/Release/sbosvr.ini" "$DATA/SboSvr.ini"
echo "[server] データフォルダを作りました: $DATA"
