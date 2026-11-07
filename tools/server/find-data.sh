#!/bin/bash
# ===========================================================================
# 本物のゲームデータ（非公開リポジトリの Release/SBODATA）の場所を探して表示する
#
#   使い方: tools/server/find-data.sh          （見つかったフォルダを 1 行で出す）
#           tools/server/find-data.sh --repo   （非公開リポジトリのフォルダを出す）
#
# マップ・NPC・アイテムなどの本物のデータはネタバレになるので、公開リポジトリには
# 10×10 の草原だけの最小データ（Release/SBODATA）しか置いていない。本物は
# 非公開リポジトリ uraraworks/SBO2003-private の Release/SBODATA にある。
#
# 探す順番:
#   1. 環境変数 SBO2003_PRIVATE_DIR（非公開リポジトリのクローンのフォルダ）
#   2. このリポジトリと同じ階層にある SBO2003-private / sbo2003-private
#      （クラウドの各スレッドでは /home/user/sbo2003-private にクローンされる）
# 見つからなければ何も出さずに終了コード 1。
# ===========================================================================
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
PARENT="$(dirname "$ROOT")"

CANDIDATES=()
if [ -n "${SBO2003_PRIVATE_DIR:-}" ]; then
	CANDIDATES+=("$SBO2003_PRIVATE_DIR")
fi
CANDIDATES+=("$PARENT/SBO2003-private" "$PARENT/sbo2003-private" "$PARENT/Sbo2003-private")

for d in "${CANDIDATES[@]}"; do
	if [ -f "$d/Release/SBODATA/SboDataCharInfo.dat" ]; then
		d="$(cd "$d" && pwd)"
		if [ "${1:-}" = "--repo" ]; then
			echo "$d"
		else
			echo "$d/Release/SBODATA"
		fi
		exit 0
	fi
done
exit 1
