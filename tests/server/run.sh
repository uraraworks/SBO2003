#!/bin/bash
# サーバー回帰テストを作って流す。使い方: tests/server/run.sh [出力ファイル]
#   出力ファイルを省略すると標準出力。BENCH 行は計測値なので比較から外す（compare.sh 参照）。
set -e
cd "$(dirname "$0")/../.."
source "${EMSDK_ENV:-$HOME/emsdk/emsdk_env.sh}" >/dev/null 2>&1
cmake --preset mac-release >/dev/null
cmake --build --preset mac-release --target SboSvrTests -j8 2>&1 | grep -E "error|Error" || true
# 実データは起動時に UTF-8 へ書き換わるので、毎回コピーに対して流す
WORK="${TMPDIR:-/tmp}/sbosvr-test-data"
rm -rf "$WORK"; mkdir -p "$WORK"
cp -R Release/SBODATA Release/sbosvr.ini "$WORK/"
if [ -n "$1" ]; then
	node out/server-release/SboSvrTests.js "$WORK" > "$1" 2>&1
else
	node out/server-release/SboSvrTests.js "$WORK" 2>&1
fi
