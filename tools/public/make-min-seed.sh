#!/bin/bash
# ===========================================================================
# 公開リポジトリ用の「最小の初期データ」（10×10 の草原だけ）を作る
#
#   使い方: tools/public/make-min-seed.sh [元の SBODATA フォルダ] [出力先]
#     元の省略時は Release/SBODATA、出力先の省略時は Release/SBODATA
#
# 本物のゲームデータ（マップ・NPC・アイテム・職業など）はネタバレになるので
# 非公開リポジトリ（uraraworks/SBO2003-private）にだけ置く。公開リポジトリには、
# 絵の並べ方の定義（マップパーツ・エフェクト・噴き出し）だけを元のデータから持ってきて、
# 残りはサーバーが自分で作る中身（マップが無い時の 10×10 の草原など）にする。
# ===========================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="${1:-$ROOT/Release/SBODATA}"
OUT="${2:-$ROOT/Release/SBODATA}"
JS="$ROOT/out/server/SboSvr.js"

if [ ! -f "$JS" ]; then
	echo "[min-seed] $JS がありません。先に tools/server/build.sh でビルドしてください" >&2
	exit 1
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
mkdir -p "$WORK/SBODATA"
# 絵の定義だけ持ってくる（ゲームの中身ではないので公開してよいもの）
for f in MapPartsInfo EfcInfo ComEfcInfo; do
	cp "$SRC/SboData$f.dat" "$WORK/SBODATA/"
done
# 待ち受けがほかのサーバーとぶつからないように、使っていなさそうなポートにする
printf '[Port]\r\nPortNo=2995\r\n[Administrator]\r\nAccount=administrator\r\nPassword=\r\n' > "$WORK/SboSvr.ini"

# サーバーを起動して、足りないデータを作らせてから保存して止める
node "$JS" --headless --data "$WORK" --http-port 0 > "$WORK/log.txt" 2>&1 &
PID=$!
for i in $(seq 1 60); do
	grep -q "待ち受けを開始しました" "$WORK/log.txt" && break
	sleep 1
done
kill -TERM "$PID"
wait "$PID" || true
cat "$WORK/log.txt"
[ -f "$WORK/SBODATA/SboDataMapInfo000.dat" ] || { echo "[min-seed] マップが作られていません" >&2; exit 1; }

# 出力先を入れ替える（預かり所はプレイヤーのものなので入れない）
mkdir -p "$OUT"
find "$OUT" -maxdepth 1 -type f -name 'SboData*' -delete
rm -f "$WORK/SBODATA/SboDataLeaveInfo.dat"
cp "$WORK/SBODATA"/SboData* "$OUT/"
echo "[min-seed] 最小の初期データを作りました: $OUT"
ls "$OUT"
