#!/bin/bash
# ===========================================================================
# プレイヤーのデータを含まない「初期データ」を作る（公開リポジトリ用）
#
#   使い方: tools/server/make-seed.sh <元の SBODATA フォルダ> [出力先]
#     出力先の省略時は Release/SBODATA
#
# 元のデータをいったん作業フォルダへコピーし、サーバーを --save-npc-only で
# 起動・終了させて NPC だけを保存し直す。そのキャラ情報と、ゲームの中身の
# ファイル（マップ・アイテムなど。元のまま）だけを出力先へコピーする。
# 預かり所（SboDataLeaveInfo.dat）やバックアップ、サーバー情報のページなど、
# プレイヤーに関わるファイルは出力しない。
# ===========================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SRC="${1:?元の SBODATA フォルダを指定してください}"
OUT="${2:-$ROOT/Release/SBODATA}"
JS="$ROOT/out/server/SboSvr.js"

if [ ! -f "$JS" ]; then
	echo "[seed] $JS がありません。先に tools/server/build.sh でビルドしてください" >&2
	exit 1
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
mkdir -p "$WORK/SBODATA"
cp -R "$SRC"/. "$WORK/SBODATA/"
# 待ち受けがほかのサーバーとぶつからないように、使っていなさそうなポートにする
printf '[Port]\r\nPortNo=2994\r\n[Administrator]\r\nAccount=administrator\r\nPassword=\r\n' > "$WORK/SboSvr.ini"

node "$JS" --headless --data "$WORK" --http-port 0 --save-npc-only > "$WORK/log.txt" 2>&1 &
PID=$!
for i in $(seq 1 60); do
	grep -q "待ち受けを開始しました" "$WORK/log.txt" && break
	sleep 1
done
kill -TERM "$PID"
wait "$PID" || true
cat "$WORK/log.txt"

mkdir -p "$OUT"
# キャラ情報だけ NPC のみで保存し直したものを使い、ほかのゲームの中身は元のファイルをそのまま使う
# （ファイル名の決め打ち。増えたらここへ足す）
cp "$WORK/SBODATA/SboDataCharInfo.dat" "$OUT/"
for f in ComEfcInfo EfcInfo EquUserInfo ItemInfo ItemInfoTest JobInfo MagicInfo MagicUserInfo MapPartsInfo StTmpInfo SubGameInfo; do
	if [ -f "$SRC/SboData$f.dat" ]; then
		cp "$SRC/SboData$f.dat" "$OUT/"
	fi
done
cp "$SRC"/SboDataMapInfo*.dat "$OUT/"
echo "[seed] 初期データを作りました: $OUT"
