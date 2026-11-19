#!/bin/bash
# 現在のコードの出力が golden.txt と一致するか確かめる（BENCH 行と [utf8] ログは除く）
set -euo pipefail
cd "$(dirname "$0")"
./run.sh "${TMPDIR:-/tmp}/sbosvr-test-now.txt"
f() { grep -v -e '^BENCH' -e '^\[' "$1"; }
if diff <(f golden.txt) <(f "${TMPDIR:-/tmp}/sbosvr-test-now.txt") > "${TMPDIR:-/tmp}/sbosvr-test-diff.txt"; then
	echo "一致"; grep '^BENCH' "${TMPDIR:-/tmp}/sbosvr-test-now.txt"
else
	echo "不一致"; head -20 "${TMPDIR:-/tmp}/sbosvr-test-diff.txt"; exit 1
fi
