#!/usr/bin/env bash
# Emscripten ports(SDL2 / SDL2_ttf / SDL2_mixer / freetype / harfbuzz / zlib)を git clone で取り込む。
# クラウドセッションのようにアーカイブ(archive/*.zip・releases/download)の直接ダウンロードが
# 塞がれていて、git clone だけ通る環境向けの回避策。普通の環境では不要(em++ が自動で取りに行く)。
#
# 使い方: source <emsdk>/emsdk_env.sh してから tools/emscripten/prefetch-ports-via-git.sh
#
# em++ は <cache>/ports/<name>/.emscripten_url が期待 URL と一致すれば取得済みとみなすので、
# 同じディレクトリ構成(<name>/<アーカイブ内のトップディレクトリ>)に clone して目印を書く。
# アーカイブと git の中身が同じ前提(sha512 は検証されない点に注意)。
set -euo pipefail

command -v em++ >/dev/null 2>&1 || { echo "em++ が見つかりません。emsdk_env.sh を読み込んでください。" >&2; exit 1; }
EMSCRIPTEN_DIR="$(dirname "$(readlink -f "$(command -v em++)")")"

# ports の定義(バージョン・URL)は emscripten 本体から読む。版が上がっても追従できるように
python3 - "$EMSCRIPTEN_DIR" <<'PY' | while read -r name url repo ref subdir portsdir; do
import sys, os
sys.path.insert(0, sys.argv[1])
from tools import cache
from tools.ports import zlib, sdl2, sdl2_ttf, sdl2_mixer, freetype, harfbuzz
ports_dir = cache.get_path('ports')
rows = [
    ('zlib', f'https://github.com/madler/zlib/archive/refs/tags/v{zlib.VERSION}.tar.gz',
     'https://github.com/madler/zlib', f'v{zlib.VERSION}', f'zlib-{zlib.VERSION}'),
    ('sdl2', f'https://github.com/libsdl-org/SDL/archive/{sdl2.TAG}.zip',
     'https://github.com/libsdl-org/SDL', sdl2.TAG, sdl2.SUBDIR),
    ('sdl2_ttf', f'https://github.com/libsdl-org/SDL_ttf/archive/{sdl2_ttf.TAG}.zip',
     'https://github.com/libsdl-org/SDL_ttf', sdl2_ttf.TAG, f'SDL_ttf-{sdl2_ttf.TAG}'),
    ('sdl2_mixer', f'https://github.com/libsdl-org/SDL_mixer/archive/{sdl2_mixer.TAG}.zip',
     'https://github.com/libsdl-org/SDL_mixer', sdl2_mixer.TAG, f'SDL_mixer-{sdl2_mixer.TAG}'),
    ('freetype', f'https://github.com/freetype/freetype/archive/{freetype.TAG}.zip',
     'https://github.com/freetype/freetype', freetype.TAG, f'freetype-{freetype.TAG}'),
    ('harfbuzz', f'https://github.com/harfbuzz/harfbuzz/releases/download/{harfbuzz.VERSION}/harfbuzz-{harfbuzz.VERSION}.tar.xz',
     'https://github.com/harfbuzz/harfbuzz', harfbuzz.VERSION, f'harfbuzz-{harfbuzz.VERSION}'),
]
for r in rows:
    print(*r, ports_dir)
PY
    dest="$portsdir/$name"
    if [ -f "$dest/.emscripten_url" ] && [ "$(cat "$dest/.emscripten_url")" = "$url" ]; then
        echo "[prefetch-ports] $name: 取得済み"
        continue
    fi
    echo "[prefetch-ports] $name: $repo ($ref) を clone"
    rm -rf "$dest" "$dest.tmp"
    mkdir -p "$dest.tmp"
    git -c advice.detachedHead=false clone --quiet --depth 1 --branch "$ref" "$repo" "$dest.tmp/$subdir"
    rm -rf "$dest.tmp/$subdir/.git"
    echo "$url" > "$dest.tmp/.emscripten_url"
    mv "$dest.tmp" "$dest"
done
echo "[prefetch-ports] done"
