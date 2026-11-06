#!/bin/bash
# 現在のコードの出力が golden.txt と一致するか確かめる（BENCH 行と [utf8] ログは除く）
cd "$(dirname "$0")"
./run.sh /tmp/sbosvr-test-now.txt
f() { grep -v -e '^BENCH' -e '^\[' "$1"; }
if diff <(f golden.txt) <(f /tmp/sbosvr-test-now.txt) > /tmp/sbosvr-test-diff.txt; then
	echo "一致"; grep '^BENCH' /tmp/sbosvr-test-now.txt
else
	echo "不一致"; head -20 /tmp/sbosvr-test-diff.txt; exit 1
fi
