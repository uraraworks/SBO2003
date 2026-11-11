#!/bin/bash
# ===========================================================================
# 非公開リポジトリ（uraraworks/SBO2003-private）から、公開できない素材（BGM の MIDI と音声ファイル）を取ってくる
#
#   使い方: SBO2003_PRIVATE_TOKEN=<トークン> tools/public/fetch-private-assets.sh
#
# 公開リポジトリの Actions（ステージング・本番のビルド）で使う。トークンは
# SBO2003-private の Contents を読めるもの（fine-grained の Personal access token など）。
# すでに SboSoundData/MIDI と SboSoundData/Audio がある時（非公開リポジトリそのもの）や、
# トークンが無い時は何もしない（MIDI・音声無しでもビルドでき、BGM が鳴らないだけ）。
# 片方だけ無い時は、無い方だけ取ってくる。
# ===========================================================================
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
REPO="${SBO2003_PRIVATE_REPO:-uraraworks/SBO2003-private}"

# 取ってくるフォルダ（SboSoundData/ の下）。もう手元にあるものは取らない
NEED=()
for d in MIDI Audio; do
	if [ -d "$ROOT/SboSoundData/$d" ]; then
		echo "[private] SboSoundData/$d はもうあります"
	else
		NEED+=("SboSoundData/$d")
	fi
done
if [ "${#NEED[@]}" = 0 ]; then
	exit 0
fi
if [ -z "${SBO2003_PRIVATE_TOKEN:-}" ]; then
	echo "[private] SBO2003_PRIVATE_TOKEN が無いので、MIDI・音声無し（BGM 無し）でビルドします"
	exit 0
fi

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
# 必要なフォルダだけを取る（トークンは URL ではなくヘッダーで渡し、ログや .git に残さない）
AUTH="Authorization: Basic $(printf 'x-access-token:%s' "$SBO2003_PRIVATE_TOKEN" | base64 -w0)"
# 公開側と同じ名前のブランチが非公開側にもあればそれを取る（素材の追加と仕組みを組で試す時、
# 非公開側をマージする前にステージングへ出せるように）。無ければ既定のブランチ。
# SBO2003_PRIVATE_REF で明示もできる。Actions では GITHUB_REF_NAME が実行したブランチ名。
REF="${SBO2003_PRIVATE_REF:-}"
if [ -z "$REF" ] && [ -n "${GITHUB_REF_NAME:-}" ] && [ "$GITHUB_REF_NAME" != "main" ]; then
	if git -c http.extraHeader="$AUTH" ls-remote --exit-code --heads "https://github.com/$REPO.git" "$GITHUB_REF_NAME" >/dev/null 2>&1; then
		REF="$GITHUB_REF_NAME"
	fi
fi
BRANCH_OPT=()
if [ -n "$REF" ]; then
	echo "[private] 非公開リポジトリのブランチ $REF から取ります"
	BRANCH_OPT=(--branch "$REF")
fi
git -c http.extraHeader="$AUTH" clone --quiet --depth 1 --filter=blob:none --sparse "${BRANCH_OPT[@]}" \
	"https://github.com/$REPO.git" "$WORK/repo"
git -C "$WORK/repo" -c http.extraHeader="$AUTH" sparse-checkout set "${NEED[@]}"
for d in "${NEED[@]}"; do
	# 非公開側にまだ無いフォルダ（音声をまだ置いていない時など）は飛ばす
	if [ -d "$WORK/repo/$d" ]; then
		cp -R "$WORK/repo/$d" "$ROOT/$d"
	else
		echo "[private] 非公開リポジトリに $d はありません"
	fi
done
[ -d "$ROOT/SboSoundData/MIDI" ] && echo "[private] MIDI: $(find "$ROOT/SboSoundData/MIDI" -iname '*.mid' | wc -l) 曲"
[ -d "$ROOT/SboSoundData/Audio" ] && echo "[private] 音声: $(find "$ROOT/SboSoundData/Audio" -iname '*.m4a' | wc -l) 曲"
exit 0
