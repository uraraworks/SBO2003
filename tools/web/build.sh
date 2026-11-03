#!/bin/bash
# ===========================================================================
# SBO2003 クライアントのブラウザ版をビルドする
#
#   使い方: tools/web/build.sh [debug|release]   （省略時は release）
#   出力先: out/web/dist/sbocli.html（+ .js / .wasm / .data）
#
# 前提: Emscripten（emsdk）が使えること。~/emsdk があれば自動で読み込む。
#
# 流れ
#   1. ソース（UTF-8）を前処理して out/web/src へ置く
#      （文字列リテラルを SJIS の 8 進エスケープにする。実行時の文字コードは SJIS のまま）
#   2. リソース DLL の .rc からリソース表を作る
#   3. 並列にコンパイルしてリンクする
# ===========================================================================
set -euo pipefail

MODE="${1:-release}"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$ROOT/out/web"
SRC="$OUT/src"
OBJ="$OUT/obj-$MODE"
DIST="$OUT/dist"
GEN="$OUT/gen"

if ! command -v em++ >/dev/null 2>&1; then
	if [ -f "$HOME/emsdk/emsdk_env.sh" ]; then
		# shellcheck disable=SC1091
		source "$HOME/emsdk/emsdk_env.sh" >/dev/null 2>&1
	else
		echo "[build] em++ が見つかりません。emsdk を用意してください" >&2
		exit 1
	fi
fi

mkdir -p "$OBJ" "$DIST" "$GEN"

echo "[build] 1/3 前処理"
python3 "$ROOT/tools/web/sjis_prepass.py" -I SboCli "$ROOT" "$SRC" SboCli common SboSoundData SboGrpData >/dev/null

echo "[build] 2/3 リソース表"
python3 "$ROOT/tools/web/gen_resources.py" "$ROOT" "$GEN"

# ---------------------------------------------------------------------------
# コンパイルするソース
#   ゲーム側は tools/web/sources.txt に載っているもの。それに互換層と
#   Web 版の差し替え実装を足す
# ---------------------------------------------------------------------------
# macOS 標準の bash 3.2 には mapfile が無いので while read で読む
GAME_SRCS=()
while IFS= read -r line; do
	case "$line" in
		''|'#'*) ;;
		*) GAME_SRCS+=("$line") ;;
	esac
done < <(tr -d '\r' < "$ROOT/tools/web/sources.txt")
WEB_SRCS=(
	"$ROOT/platform/w32_core.cpp"
	"$ROOT/platform/w32_gdi.cpp"
	"$ROOT/platform/w32_ctrl.cpp"
	"$ROOT/platform/w32_dlg.cpp"
	"$ROOT/platform/w32_mfc.cpp"
	"$ROOT/platform/w32_sjis.cpp"
	"$ROOT/platform/w32_file.cpp"
	"$ROOT/web/w32_html.cpp"
	"$ROOT/web/w32_perf.cpp"
	"$ROOT/web/web_main.cpp"
	"$ROOT/web/replace/CDXAudio_web.cpp"
	"$ROOT/web/replace/LibMusicLoader_web.cpp"
	"$ROOT/web/replace/MacAddr_web.cpp"
	"$ROOT/web/replace/CUraraSockTCP_web.cpp"
	"$ROOT/web/replace/SboCliMsgLogWindow_web.cpp"
	"$ROOT/web/replace/SboCliWebAccount_web.cpp"
	"$GEN/w32_resources.cpp"
)

PORTS=(-sUSE_SDL=2 -sUSE_SDL_TTF=2 -sUSE_SDL_MIXER=2 -sUSE_LIBPNG=1 -sUSE_ZLIB=1)
if [ "$MODE" = "debug" ]; then
	OPT=(-O0 -g)
else
	OPT=(-O2)
fi
CXXFLAGS=(-std=gnu++17 -fms-extensions -fno-strict-aliasing -w "${OPT[@]}" "${PORTS[@]}"
	-I "$ROOT/platform/include"
	-I "$SRC" -I "$SRC/SboCli" -I "$SRC/SboCli/src" -I "$SRC/common")
GAME_FLAGS=(-include "$SRC/SboCli/StdAfx.h")

echo "[build] 3/3 コンパイル（${#GAME_SRCS[@]} + ${#WEB_SRCS[@]} ファイル）"
JOBS="$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"
OBJS=()
compile_list=()
for s in "${GAME_SRCS[@]}"; do
	o="$OBJ/$(echo "$s" | tr '/' '_').o"
	OBJS+=("$o")
	compile_list+=("$SRC/$s|$o|game")
done
for s in "${WEB_SRCS[@]}"; do
	o="$OBJ/web_$(basename "$s").o"
	OBJS+=("$o")
	case "$s" in
		*/replace/*) compile_list+=("$s|$o|game") ;;
		*)           compile_list+=("$s|$o|web") ;;
	esac
done

compile_one() {
	local src="${1%%|*}" rest="${1#*|}"
	local obj="${rest%%|*}" kind="${rest#*|}"
	# ソースかヘッダが新しい（または無くなった）ならコンパイルし直す（依存は .d で見る）
	if [ -f "$obj" ] && [ -f "$obj.d" ]; then
		local newer
		newer="$(sed 's/\\$//' "$obj.d" | tr ' ' '\n' | grep -v ':$' | grep -v '^$' | while read -r f; do { [ ! -e "$f" ] || [ "$f" -nt "$obj" ]; } && echo x && break; done || true)"
		[ -z "$newer" ] && return 0
	fi
	local extra=()
	[ "$kind" = "game" ] && extra=("${GAME_FLAGS[@]}")
	em++ "${CXXFLAGS[@]}" ${extra[@]+"${extra[@]}"} -MMD -MF "$obj.d" -c "$src" -o "$obj"
}
fail=0
# macOS 標準の bash 3.2 には wait -n が無いので、JOBS 個ずつまとめて待つ
pids=()
for item in "${compile_list[@]}"; do
	compile_one "$item" &
	pids+=($!)
	if [ "${#pids[@]}" -ge "$JOBS" ]; then
		for p in "${pids[@]}"; do
			wait "$p" || fail=1
		done
		pids=()
	fi
done
for p in "${pids[@]+"${pids[@]}"}"; do
	wait "$p" || fail=1
done
if [ "$fail" -ne 0 ]; then
	echo "[build] コンパイルに失敗しました" >&2
	exit 1
fi

echo "[build] リンク"
# SBO_LDFLAGS: 計測用にリンクへ足すオプション（例: SBO_LDFLAGS=--profiling-funcs で関数名を残す）
# --use-preload-cache: .data をブラウザの IndexedDB にも保存し、中身が同じ間は次からそちらを使う
PRELOAD=(--preload-file "$ROOT/web/font@/font")
while IFS= read -r line; do
	[ -n "$line" ] && PRELOAD+=(--preload-file "$line")
done < "$GEN/preload.txt"

em++ "${OPT[@]}" ${SBO_LDFLAGS:-} "${PORTS[@]}" "${OBJS[@]}" -o "$DIST/sbocli.html" \
	--shell-file "$ROOT/web/shell/sbocli.html" \
	"${PRELOAD[@]}" --use-preload-cache \
	-lidbfs.js -lwebsocket.js \
	-sGLOBAL_BASE=1048576 \
	-sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=64MB -sSTACK_SIZE=1MB \
	-sEXPORTED_RUNTIME_METHODS=FS,UTF8ToString,stringToNewUTF8 \
	-sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE='$stringToNewUTF8,$UTF8ToString' \
	-sEXPORTED_FUNCTIONS=_main,_malloc,_free,_W32JsOnKey,_W32JsOnMouse,_W32JsOnActivate,_W32JsOnButton

# フッターに反映した日時とコミットを書き込む（SBOP2 と同じく、どの版が動いているか分かるように）。
# Docker の中など git が使えない所では、環境変数 SBO2003_BUILD_COMMIT で渡す。
BUILD_COMMIT="${SBO2003_BUILD_COMMIT:-$(git -C "$ROOT" rev-parse --short HEAD 2>/dev/null || true)}"
BUILD_COMMIT="${BUILD_COMMIT:-unknown}"
BUILD_DATE="${SBO2003_BUILD_DATE:-$(TZ=Asia/Tokyo date '+%Y-%m-%d %H:%M')}"
# （macOS の sed は -i の書き方が違うので、一時ファイルを経由する）
sed "s/@@SBO_BUILD_DATE@@/$BUILD_DATE/g; s/@@SBO_BUILD_COMMIT@@/$BUILD_COMMIT/g" "$DIST/sbocli.html" > "$DIST/sbocli.html.tmp"
mv "$DIST/sbocli.html.tmp" "$DIST/sbocli.html"
echo "[build] 版: $BUILD_DATE $BUILD_COMMIT"

# BGM（MIDI）用の JS シンセとサウンドフォントは、ページから後で読み込むので .data に入れずに置く
mkdir -p "$DIST/bgm"
cp "$ROOT/web/bgm/sbo_bgm.js" \
	"$ROOT/web/vendor/spessasynth/spessasynth.js" \
	"$ROOT/web/vendor/spessasynth/spessasynth_processor.min.js" \
	"$ROOT/web/soundfont/GeneralUserGS.sf3" \
	"$DIST/bgm/"

# アカウント復旧の承認ページ（管理者用。サーバーの /api/admin/* を使う）
cp "$ROOT/web/account/recover-admin.html" "$DIST/"

echo "[build] 完了: $DIST/sbocli.html"
