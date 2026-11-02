#!/bin/bash
# ===========================================================================
# BGM 再生に使う spessasynth_lib（MIDI シンセ）を web/vendor/spessasynth に取り込む
#
#   使い方: tools/web/update_spessasynth.sh [バージョン]   （省略時は下の既定）
#
# 取り込んだファイルはリポジトリに入れておくので、普段のビルドでは
# npm もネットワークも要らない。バージョンを上げたいときだけ実行する。
#
#   spessasynth.js                 本体（spessasynth_core も含めて 1 ファイルにまとめたもの）
#   spessasynth_processor.min.js   AudioWorklet で動く音源部分（配布物そのまま）
# ===========================================================================
set -euo pipefail

VER="${1:-4.3.14}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
DEST="$ROOT/web/vendor/spessasynth"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

cd "$WORK"
npm init -y >/dev/null
npm install --no-audit --no-fund "spessasynth_lib@$VER" esbuild >/dev/null
echo 'export * from "spessasynth_lib";' > entry.js
npx esbuild entry.js --bundle --format=esm --minify --legal-comments=eof --outfile=spessasynth.js

mkdir -p "$DEST"
cp spessasynth.js "$DEST/"
cp node_modules/spessasynth_lib/dist/spessasynth_processor.min.js "$DEST/"
cp node_modules/spessasynth_lib/LICENSE "$DEST/LICENSE"
echo "spessasynth_lib $(node -p 'require("spessasynth_lib/package.json").version'), spessasynth_core $(node -p 'require("spessasynth_core/package.json").version')" > "$DEST/VERSION.txt"
echo "[spessasynth] 取り込みました: $(cat "$DEST/VERSION.txt")"
