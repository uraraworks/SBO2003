#!/bin/bash
# ===========================================================================
# サーバー（SboSvr）の Node.js 版をビルドする
#
#   使い方: tools/server/build.sh [debug|release]   （省略時は debug）
#   出力先: out/server/SboSvr.js（release は out/server-release/）
#
# 前提: Emscripten（emsdk）と CMake。~/emsdk があれば自動で読み込む。
# ===========================================================================
set -euo pipefail

MODE="${1:-debug}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

if [ -z "${EMSDK:-}" ] || ! command -v em++ >/dev/null 2>&1; then
	if [ -f "$HOME/emsdk/emsdk_env.sh" ]; then
		# shellcheck disable=SC1091
		source "$HOME/emsdk/emsdk_env.sh" >/dev/null 2>&1
	else
		echo "[server] emsdk が見つかりません。docs/vscode-mac.md の手順で用意してください" >&2
		exit 1
	fi
fi

case "$MODE" in
	debug)   PRESET=mac-debug ;;
	release) PRESET=mac-release ;;
	*) echo "使い方: $0 [debug|release]" >&2; exit 1 ;;
esac

JOBS="$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"
cmake --preset "$PRESET" >/dev/null
cmake --build --preset "$PRESET" -j "$JOBS"
