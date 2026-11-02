#!/usr/bin/env bash
# Sbo2003 本番（Windows Server の VPS）用の配布 zip を作る。
#
#   deploy/production/make-zip.sh <サーバーのフォルダ> <ブラウザ版のフォルダ> <出力先フォルダ>
#
#   サーバーのフォルダ    tools/server/build.sh release の出力（out/server-release）
#   ブラウザ版のフォルダ  tools/web/build.sh release の出力（out/web/dist）
#
# 出力: <出力先>/Sbo2003_Server_yyyyMMdd_HHmmss.zip（日時は日本時間）
# Windows 用の Node.js（node.exe）も同梱するので、VPS に Node.js を入れておく必要はない。
# 遊んだデータ（data\）と設定（sbo2003.env）は zip に入れないので、上書き展開しても消えない。
set -euo pipefail

# 同梱する Node.js（ステージングの node:22 と同じ系統。公開から 2 週間以上たった版に固定する）
NODE_VERSION=v22.23.3

SERVER_DIR="$(cd "${1:?サーバーのフォルダを指定してください}" && pwd)"
WEB_DIR="$(cd "${2:?ブラウザ版のフォルダを指定してください}" && pwd)"
OUT_DIR="${3:?出力先フォルダを指定してください}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
HERE="$ROOT/deploy/production"

[ -f "$SERVER_DIR/SboSvr.js" ] || { echo "SboSvr.js がありません: $SERVER_DIR" >&2; exit 1; }
[ -f "$WEB_DIR/sbocli.html" ] || { echo "sbocli.html がありません: $WEB_DIR" >&2; exit 1; }

mkdir -p "$OUT_DIR"
OUT_DIR="$(cd "$OUT_DIR" && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
STAGE="$WORK/Sbo2003"
mkdir -p "$STAGE/server" "$STAGE/web" "$STAGE/seed" "$STAGE/node" "$STAGE/iis"

echo "[zip] サーバーとブラウザ版をまとめます"
cp -R "$SERVER_DIR/." "$STAGE/server/"
cp -R "$WEB_DIR/." "$STAGE/web/"
# 初回起動時に data\ へコピーする元データ（ステージングの seed と同じ）
cp -R "$ROOT/Release/SBODATA" "$STAGE/seed/SBODATA"
cp "$ROOT/Release/sbosvr.ini" "$STAGE/seed/SboSvr.ini"

# VPS で使うスクリプト類（.ps1 は Windows PowerShell 5.1 が日本語を読めるよう BOM 付き UTF-8 で置く）
for f in start.ps1 stop.ps1 install-task.ps1; do
	printf '\xEF\xBB\xBF' > "$STAGE/$f"
	sed 's/$/\r/' "$HERE/$f" >> "$STAGE/$f"
done
cp "$HERE/set-admin-password.js" "$STAGE/"
sed 's/$/\r/' "$HERE/sbo2003.env.example" > "$STAGE/sbo2003.env.example"
sed 's/$/\r/' "$HERE/web.config" > "$STAGE/iis/web.config"

echo "[zip] Node.js $NODE_VERSION（Windows x64）を取ってきます"
NODE_ZIP="node-$NODE_VERSION-win-x64.zip"
curl -fsSL -o "$WORK/$NODE_ZIP" "https://nodejs.org/dist/$NODE_VERSION/$NODE_ZIP"
curl -fsSL -o "$WORK/SHASUMS256.txt" "https://nodejs.org/dist/$NODE_VERSION/SHASUMS256.txt"
# 公式のハッシュと照らし合わせる（壊れた・すり替えられたファイルを同梱しない）
(cd "$WORK" && grep " $NODE_ZIP\$" SHASUMS256.txt | sha256sum -c -)
unzip -q -j "$WORK/$NODE_ZIP" "node-$NODE_VERSION-win-x64/node.exe" "node-$NODE_VERSION-win-x64/LICENSE" -d "$STAGE/node"

STAMP="$(TZ=Asia/Tokyo date +%Y%m%d_%H%M%S)"
COMMIT="$(git -C "$ROOT" rev-parse --short HEAD 2>/dev/null || echo unknown)"
{
	sed "s/@@STAMP@@/$STAMP/; s/@@COMMIT@@/$COMMIT/; s/@@NODE_VERSION@@/$NODE_VERSION/" "$HERE/README.txt"
} | sed 's/$/\r/' > "$STAGE/README.txt"

ZIP="$OUT_DIR/Sbo2003_Server_$STAMP.zip"
(cd "$STAGE" && zip -q -r "$ZIP" .)
echo "[zip] 作りました: $ZIP（$(du -h "$ZIP" | cut -f1)）"
