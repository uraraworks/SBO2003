#!/bin/bash
# ===========================================================================
# 非公開リポジトリ（uraraworks/SBO2003-private）から、公開できない素材（BGM の MIDI）を取ってくる
#
#   使い方: SBO2003_PRIVATE_TOKEN=<トークン> tools/public/fetch-private-assets.sh
#
# 公開リポジトリの Actions（ステージング・本番のビルド）で使う。トークンは
# SBO2003-private の Contents を読めるもの（fine-grained の Personal access token など）。
# すでに SboSoundData/MIDI がある時（非公開リポジトリそのもの）や、トークンが
# 無い時は何もしない（MIDI 無しでもビルドでき、BGM が鳴らないだけ）。
# ===========================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
REPO="${SBO2003_PRIVATE_REPO:-uraraworks/SBO2003-private}"

if [ -d "$ROOT/SboSoundData/MIDI" ]; then
	echo "[private] SboSoundData/MIDI はもうあります"
	exit 0
fi
if [ -z "${SBO2003_PRIVATE_TOKEN:-}" ]; then
	echo "[private] SBO2003_PRIVATE_TOKEN が無いので、MIDI 無し（BGM 無し）でビルドします"
	exit 0
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
# 必要なフォルダだけを取る（トークンは URL ではなくヘッダーで渡し、ログや .git に残さない）
AUTH="Authorization: Basic $(printf 'x-access-token:%s' "$SBO2003_PRIVATE_TOKEN" | base64 -w0)"
git -c http.extraHeader="$AUTH" clone --quiet --depth 1 --filter=blob:none --sparse \
	"https://github.com/$REPO.git" "$WORK/repo"
git -C "$WORK/repo" -c http.extraHeader="$AUTH" sparse-checkout set SboSoundData/MIDI
cp -R "$WORK/repo/SboSoundData/MIDI" "$ROOT/SboSoundData/MIDI"
echo "[private] MIDI を取ってきました: $(find "$ROOT/SboSoundData/MIDI" -iname '*.mid' | wc -l) 曲"
