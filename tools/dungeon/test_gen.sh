#!/bin/bash
# DungeonGen の単体テストをホストの g++ でビルドして流す（-v で格子も表示）
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${TMPDIR:-/tmp}/sbo_dungeon_test"
g++ -std=c++17 -O1 -g -Wall -Wextra -fsanitize=address,undefined -I"$ROOT" \
	"$ROOT"/SboSvr/src/DungeonGen/*.cpp "$ROOT/tools/dungeon/test_gen.cpp" -o "$OUT"
"$OUT" "$@"
