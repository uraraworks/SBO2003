#!/bin/bash
# ===========================================================================
# tools/server/run.sh で起動したサーバーを止める（データを保存してから終わる）
# ===========================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
DATA="${SBO_DATA:-$ROOT/out/server-data}"
PIDFILE="$DATA/SboSvr.pid"

if [ ! -f "$PIDFILE" ]; then
	echo "[server] 動いているサーバーが見つかりません"
	exit 0
fi
PID="$(cat "$PIDFILE")"
if ! kill -0 "$PID" 2>/dev/null; then
	echo "[server] 動いているサーバーが見つかりません"
	rm -f "$PIDFILE"
	exit 0
fi
kill -INT "$PID"
# 保存が終わるまで待つ（最大 30 秒）
for _ in $(seq 1 60); do
	if ! kill -0 "$PID" 2>/dev/null; then
		rm -f "$PIDFILE"
		echo "[server] 停止しました"
		exit 0
	fi
	sleep 0.5
done
echo "[server] 30 秒待っても止まりませんでした（プロセス番号 $PID）" >&2
exit 1
