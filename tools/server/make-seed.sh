#!/bin/bash
# ===========================================================================
# プレイヤーのデータを含まない「初期データ」を作る（非公開リポジトリの Release/SBODATA 用）
#
#   使い方: tools/server/make-seed.sh <元の SBODATA フォルダ> [出力先]
#     出力先の省略時は、非公開リポジトリ（SBO2003-private）のクローンの Release/SBODATA
#     （探し方は tools/server/find-data.sh）
#
# マップ・NPC などの本物のデータはネタバレになるので、公開リポジトリの Release/SBODATA
# （10×10 の草原だけ。tools/public/make-min-seed.sh で作る）には出力しない。
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
if [ -n "${2:-}" ]; then
	OUT="$2"
else
	OUT="$("$ROOT/tools/server/find-data.sh")" || {
		echo "[seed] 出力先を指定するか、非公開リポジトリ（SBO2003-private）を隣にクローンしてください" >&2
		exit 1
	}
fi
case "$(mkdir -p "$OUT" && cd "$OUT" && pwd)" in
	"$ROOT/Release/SBODATA") echo "[seed] 公開リポジトリの Release/SBODATA には本物のデータを置けません（ネタバレ防止）" >&2; exit 1 ;;
esac
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
# UTF-8 の目印（SboDataUtf8.txt）。サーバーは目印が無いと .dat を Shift_JIS とみなして UTF-8 に変換し、目印を作る。
# 元が UTF-8（目印あり）ならゲームの中身のファイルは元のまま、元が Shift_JIS（目印なし）ならサーバーが変換したものを使う。
# 目印は必ず出力先へ写す（無いと、ローカルのサーバーが起動のたびにもう一度変換して文字化けする）
if [ ! -f "$WORK/SBODATA/SboDataUtf8.txt" ]; then
	echo "[seed] サーバーが UTF-8 の目印（SboDataUtf8.txt）を作っていません。上のログを確かめてください" >&2
	exit 1
fi
if [ -f "$SRC/SboDataUtf8.txt" ]; then
	FROM="$SRC"
else
	FROM="$WORK/SBODATA"
fi
cp "$WORK/SBODATA/SboDataCharInfo.dat" "$OUT/"
for f in ComEfcInfo EfcInfo EquUserInfo ItemInfo ItemInfoTest JobInfo MagicInfo MagicUserInfo MapPartsInfo StTmpInfo SubGameInfo; do
	if [ -f "$FROM/SboData$f.dat" ]; then
		cp "$FROM/SboData$f.dat" "$OUT/"
	fi
done
cp "$FROM"/SboDataMapInfo*.dat "$OUT/"
cp "$WORK/SBODATA/SboDataUtf8.txt" "$OUT/"
echo "[seed] 初期データを作りました: $OUT"
