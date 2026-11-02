#!/bin/bash
# ===========================================================================
# 公開リポジトリ用に、いまのコミットから公開できるファイルだけを書き出す
#
#   使い方: tools/public/export.sh <出力先フォルダ>
#
# このリポジトリ（非公開）には、再配布できない MIDI と、プレイヤーのデータが
# 入っている。それらを除いた中身を出力先に展開し、公開リポジトリの .gitignore
# に除いたものを足す（公開側で誤って追加しないように）。
# 出力先は空のフォルダか、公開リポジトリの作業フォルダ（.git は残す）を指定する。
# ===========================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${1:?出力先フォルダを指定してください}"

# 公開しないもの（ここを増やしたら、下の確認にも足す）
EXCLUDES=(
	"private"				# プレイヤーのデータ（本番の移行用）
	"SboSoundData/MIDI"		# 再配布できない BGM
)

mkdir -p "$OUT"
# 公開リポジトリの作業フォルダなら、.git 以外を入れ替える
find "$OUT" -mindepth 1 -maxdepth 1 ! -name .git -exec rm -rf {} +

ARGS=()
for e in "${EXCLUDES[@]}"; do
	ARGS+=(":(exclude)$e")
done
git -C "$ROOT" archive --format=tar HEAD -- . "${ARGS[@]}" | tar -x -C "$OUT"

{
	echo ""
	echo "# 公開リポジトリには入れないもの（tools/public/export.sh）"
	for e in "${EXCLUDES[@]}"; do
		echo "/$e/"
	done
} >> "$OUT/.gitignore"

# 念のための確認: 除いたはずのものと、プレイヤーのデータらしいファイルが無いこと
NG=0
for e in "${EXCLUDES[@]}"; do
	if [ -e "$OUT/$e" ]; then
		echo "[export] 残っています: $e" >&2
		NG=1
	fi
done
if find "$OUT" -iname '*.mid' -o -iname 'SboDataLeaveInfo.dat' -o -iname 'SboDataCharInfo_*.dat' | grep -q .; then
	echo "[export] MIDI かプレイヤーのデータらしいファイルが残っています" >&2
	find "$OUT" -iname '*.mid' -o -iname 'SboDataLeaveInfo.dat' -o -iname 'SboDataCharInfo_*.dat' >&2
	NG=1
fi
if [ "$NG" != 0 ]; then
	exit 1
fi
echo "[export] 書き出しました: $OUT"
