#!/bin/bash
# ===========================================================================
# サーバーの実行用フォルダを用意する
#
#   使い方: tools/server/setup-data.sh [フォルダ]   （省略時は out/server-data）
#
# ゲームデータ（SBODATA）と Release/sbosvr.ini をコピーする。すでにあれば何もしない
# （遊んだデータを上書きしないため。作り直す時はフォルダを消してから実行する）。
# SboSvr は「実行ファイル名.ini」を読むので、SboSvr.ini という名前で置く。
#
# ゲームデータは、非公開リポジトリ（SBO2003-private）のクローンが見つかれば
# その Release/SBODATA（本物のマップ・NPC など）を使う。探し方は find-data.sh。
# 見つからなければ、公開リポジトリの Release/SBODATA（10×10 の草原だけ）を使う。
# ===========================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
DATA="${1:-$ROOT/out/server-data}"

if [ -d "$DATA/SBODATA" ]; then
	echo "[server] データフォルダはもうあります: $DATA"
	exit 0
fi

if SRC="$("$ROOT/tools/server/find-data.sh")"; then
	echo "[server] 非公開リポジトリのゲームデータを使います: $SRC"
else
	SRC="$ROOT/Release/SBODATA"
	echo "[server] 非公開リポジトリが見つからないので、10×10 の草原だけの最小データを使います"
	echo "         （本物のデータで試すには SBO2003-private を隣にクローンするか、SBO2003_PRIVATE_DIR を設定）"
fi

mkdir -p "$DATA/SBODATA"
# 「SboDataMapInfo075(広い町バックアップ).dat」のような控えはサーバーが読まないので持ってこない
find "$SRC" -maxdepth 1 -type f | grep -E '/SboData[A-Za-z]+[0-9]*\.(dat|txt)$' | while IFS= read -r f; do cp "$f" "$DATA/SBODATA/"; done
cp "$ROOT/Release/sbosvr.ini" "$DATA/SboSvr.ini"
echo "[server] データフォルダを作りました: $DATA"
